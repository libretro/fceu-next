#include "libsnes.hpp"

#include <stdio.h>

static snes_video_refresh_t video_cb = NULL;
static snes_audio_sample_t audio_cb = NULL;
static snes_input_poll_t poll_cb = NULL;
static snes_input_state_t input_cb = NULL;

unsigned snes_library_revision_major(void)
{
   return 1;
}

unsigned snes_library_revision_minor(void)
{
   return 3;
}

const char *snes_library_id(void)
{
   return "FCEUNext";
}

void snes_set_video_refresh(snes_video_refresh_t cb)
{
   video_cb = cb;
}

void snes_set_audio_sample(snes_audio_sample_t cb)
{
   audio_cb = cb;
}

void snes_set_input_poll(snes_input_poll_t cb)
{
   poll_cb = cb;
}

void snes_set_input_state(snes_input_state_t cb)
{
   input_cb = cb;
}

void snes_set_controller_port_device(bool, unsigned)
{}

void snes_set_cartridge_basename(const char*)
{}

void snes_init(void) {}

static unsigned serialize_size = 0;

static void gba_init(void)
{
#if 0
   cpuSaveType = 0;
   flashSize = 0x10000;
   enableRtc = false;
   mirroringEnable = false;

   utilUpdateSystemColorMaps();

   CPUInit(0, false);
   CPUReset();

   soundInit();
   soundSetSampleRate(31900);
   soundReset();
   soundResume();

   uint8_t *state_buf = new uint8_t[2000000];

   serialize_size = CPUWriteState_libgba(state_buf, 2000000);
   delete[] state_buf;
#endif
}

void snes_term(void) {}

void snes_power(void)
{}

void snes_reset(void)
{}

void snes_run(void)
{
#if 0
   CPULoop();
   systemReadJoypadGBA();
#endif
}


unsigned snes_serialize_size(void)
{
   return serialize_size;
}

bool snes_serialize(uint8_t *data, unsigned size)
{
   //return CPUWriteState_libgba(data, size);
   return 1;
}

bool snes_unserialize(const uint8_t *data, unsigned size)
{
   //return CPUReadState_libgba(data, size);
   return 1;
}

void snes_cheat_reset(void)
{}

void snes_cheat_set(unsigned, bool, const char*)
{}

bool snes_load_cartridge_normal(const char*, const uint8_t *rom_data, unsigned rom_size)
{
   const char *tmppath = tmpnam(NULL);
   if (!tmppath)
      return false;

   FILE *file = fopen(tmppath, "wb");
   if (!file)
      return false;

   fwrite(rom_data, 1, rom_size, file);
   fclose(file);
   #if 0
   unsigned ret = CPULoadRom(tmppath);
   unlink(tmppath);

   gba_init();

   return ret;
   #endif
   return 1;
}

bool snes_load_cartridge_bsx_slotted(
  const char*, const uint8_t*, unsigned,
  const char*, const uint8_t*, unsigned
)
{ return false; }

bool snes_load_cartridge_bsx(
  const char*, const uint8_t *, unsigned,
  const char*, const uint8_t *, unsigned
)
{ return false; }

bool snes_load_cartridge_sufami_turbo(
  const char*, const uint8_t*, unsigned,
  const char*, const uint8_t*, unsigned,
  const char*, const uint8_t*, unsigned
)
{ return false; }

bool snes_load_cartridge_super_game_boy(
  const char*, const uint8_t*, unsigned,
  const char*, const uint8_t*, unsigned
)
{ return false; }

void snes_unload_cartridge(void)
{}

bool snes_get_region(void)
{
   return SNES_REGION_NTSC;
}

uint8_t *snes_get_memory_data(unsigned id)
{
   if (id != SNES_MEMORY_CARTRIDGE_RAM)
      return 0;
   /*
   if (eepromInUse)
      return eepromData;

   if (saveType == 1 || saveType == 2)
      return flashSaveMemory;

   return 0;
   */
   //return flashSaveMemory;
   return 0;
}

unsigned snes_get_memory_size(unsigned id)
{
   if (id != SNES_MEMORY_CARTRIDGE_RAM)
      return 0;

   /*
   if (eepromInUse)
      return eepromSize;

   if (saveType == 1)
      return 0x10000;
   else if (saveType == 2)
      return flashSize;
   else
      return 0;
   */
   return 0x10000;
}

void systemOnWriteDataToSoundBuffer(int16_t *finalWave, int length)
{
   for (int i = 0; i < length; i += 2)
      audio_cb(finalWave[i + 0], finalWave[i + 1]);
}
