#include <stdio.h>
#include <stdlib.h>

#include "types.h"
#include "fceu.h"

#include "driver.h"
#include "sound.h"
#include "wave.h"
#include <alloca.h>

#ifndef SN_TARGET_PS3
static FILE *soundlog=0;
#endif
static long wsize;

/* Checking whether the file exists before wiping it out is left up to the 
   reader..err...I mean, the driver code, if it feels so inclined(I don't feel
   so).
*/
void FCEU_WriteWaveData(int32 *Buffer, int Count)
{
	//mbg merge 7/17/06 changed to alloca
 //int16 temp[Count];  /* Yay.  Is this the first use of this "feature" of C in FCE Ultra? */
 int16 *temp = (int16*)alloca(Count*2);

 int16 *dest;
 int x;

#ifndef WIN32
 //if(!soundlog) return;
#else
 if(!soundlog && !FCEUI_AviIsRecording()) return;
#endif

 dest=temp;
 x=Count;

 //mbg 7/28/06 - we appear to be guaranteeing little endian
 do
 {
  x--;
  int16 tmp=*Buffer;

  *(uint8 *)dest=(((uint16)tmp)&255);
  *(((uint8 *)dest)+1)=(((uint16)tmp)>>8);
  dest++;
  Buffer++;
 }while(x);
 #ifndef SN_TARGET_PS3
 if(soundlog)
	 wsize+=fwrite(temp,1,Count*sizeof(int16),soundlog);
 #endif

	#ifdef WIN32
	if(FCEUI_AviIsRecording())
	{
		FCEUI_AviSoundUpdate((void*)temp, Count);
	}
	#endif
}

#if 0
int FCEUI_EndWaveRecord()
{
 long s;

 if(!soundlog) return 0;
 s=ftell(soundlog)-8;
 fseek(soundlog,4,SEEK_SET);
 fputc(s&0xFF,soundlog);
 fputc((s>>8)&0xFF,soundlog);
 fputc((s>>16)&0xFF,soundlog);
 fputc((s>>24)&0xFF,soundlog);
 
 fseek(soundlog,0x28,SEEK_SET);
 s=wsize;
 fputc(s&0xFF,soundlog);
 fputc((s>>8)&0xFF,soundlog);
 fputc((s>>16)&0xFF,soundlog);
 fputc((s>>24)&0xFF,soundlog);
 
 fclose(soundlog);
 soundlog=0;
 return 1;
}
#endif


#if 0
bool FCEUI_BeginWaveRecord(const char *fn)
{
 int r;

 if(!(soundlog=FCEUD_UTF8fopen(fn,"wb")))
  return false;
 wsize=0;


 /* Write the header. */
 fputs("RIFF",soundlog);
 fseek(soundlog,4,SEEK_CUR);  // Skip size
 fputs("WAVEfmt ",soundlog);

 fputc(0x10,soundlog);
 fputc(0,soundlog);
 fputc(0,soundlog);
 fputc(0,soundlog);

 fputc(1,soundlog);     // PCM
 fputc(0,soundlog);

 fputc(1,soundlog);     // Monophonic
 fputc(0,soundlog);

 r=FSettings.SndRate;
 fputc(r&0xFF,soundlog);
 fputc((r>>8)&0xFF,soundlog);
 fputc((r>>16)&0xFF,soundlog);
 fputc((r>>24)&0xFF,soundlog);
 r<<=1;
 fputc(r&0xFF,soundlog);
 fputc((r>>8)&0xFF,soundlog);
 fputc((r>>16)&0xFF,soundlog);
 fputc((r>>24)&0xFF,soundlog);
 fputc(2,soundlog);
 fputc(0,soundlog);
 fputc(16,soundlog);
 fputc(0,soundlog);
 
 fputs("data",soundlog);
 fseek(soundlog,4,SEEK_CUR);

 return true;
}
#endif
