/* FCE Ultra - NES/Famicom Emulator
*
* Copyright notice for this file:
*  Copyright (C) 2002 Xodnizel
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include <stdint.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <zlib.h>

#ifdef _USE_SHARED_MEMORY_
#include <windows.h>
#endif

#include "types.h"
#include "video.h"
#include "fceu.h"
#include "file.h"
#include "utils/memory.h"
#include "utils/crc32.h"
#include "state.h"
#include "palette.h"
#include "nsf.h"
#include "input.h"
#include "vsuni.h"
#include "drawing.h"
#include "driver.h"
#ifdef _S9XLUA_H
#include "fceulua.h"
#endif

uint8 *XBuf=NULL;
uint8 *XBackBuf=NULL;
int ClipSidesOffset=0;	//Used to move displayed messages when Clips left and right sides is checked
static uint8 *xbsave=NULL;

GUIMESSAGE guiMessage;
GUIMESSAGE subtitleMessage;

//for input display
extern int input_display;
extern uint32 cur_input_display;

bool oldInputDisplay = false;

#ifdef _USE_SHARED_MEMORY_
HANDLE mapXBuf;
#endif

std::string AsSnapshotName ="";			//adelikat:this will set the snapshot name when for s savesnapshot as function

void FCEUI_SetSnapshotAsName(std::string name) { AsSnapshotName = name; }
std::string FCEUI_GetSnapshotAsName() { return AsSnapshotName; }

void FCEU_KillVirtualVideo(void)
{
	//mbg merge TODO 7/17/06 temporarily removed
	//if(xbsave)
	//{
	// free(xbsave);
	// xbsave=0;
	//}
	//if(XBuf)
	//{
	//UnmapViewOfFile(XBuf);
	//CloseHandle(mapXBuf);
	//mapXBuf=NULL;
	//}
	//if(XBackBuf)
	//{
	// free(XBackBuf);
	// XBackBuf=0;
	//}
}

/**
* Return: Flag that indicates whether the function was succesful or not.
*
* TODO: This function is Windows-only. It should probably be moved.
**/
int FCEU_InitVirtualVideo(void)
{
	if(!XBuf)		/* Some driver code may allocate XBuf externally. */
		/* 256 bytes per scanline, * 240 scanline maximum, +16 for alignment,
		*/

#ifdef _USE_SHARED_MEMORY_

		mapXBuf  = CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,PAGE_READWRITE, 0, 256 * 256 + 16, "fceu.XBuf");

	if(mapXBuf == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(mapXBuf);
		mapXBuf = NULL;
		XBuf = (uint8*) (FCEU_malloc(256 * 256 + 16));
		XBackBuf = (uint8*) (FCEU_malloc(256 * 256 + 16));
	}
	else
	{
		XBuf = (uint8 *)MapViewOfFile(mapXBuf, FILE_MAP_WRITE, 0, 0, 0);
		XBackBuf = (uint8*) (FCEU_malloc(256 * 256 + 16));
	}

	if (!XBuf || !XBackBuf )
	{
		return 0;
	}

#else

		if(!(XBuf= (uint8*) (FCEU_malloc(256 * 256 + 16))) ||
			!(XBackBuf= (uint8*) (FCEU_malloc(256 * 256 + 16))))
		{
			return 0;
		}

#endif //_USE_SHARED_MEMORY_

		xbsave = XBuf;

		if( sizeof(uint8*) == 4 )
		{
			uintptr_t m = (uintptr_t)XBuf;
			m = ( 8 - m) & 7;
			XBuf+=m;
		}

		memset(XBuf,128,256*256); //*240);
		memset(XBackBuf,128,256*256);

		return 1;
}


#ifdef FRAMESKIP

//#define SHOWFPS
void ShowFPS(void);
#if 0
void FCEU_PutImageDummy(void)
{
	#if 0
	if(GameInfo->type!=GIT_NSF)
	{
	#endif
		FCEU_DrawNTSCControlBars(XBuf);
		FCEU_DrawSaveStates(XBuf);
		//FCEU_DrawMovies(XBuf);
	//}
	if(guiMessage.howlong) guiMessage.howlong--; /* DrawMessage() */
}
#endif
#endif

#if 0
void FCEU_PutImage(void)
{
   #if 0
	if(GameInfo->type==GIT_NSF)
	{
		DrawNSF(XBuf);

		//Save snapshot after NSF screen is drawn.  Why would we want to do it before?
		if(dosnapsave==1)
		{
			ReallySnap();
			dosnapsave=0;
		}
	} 
	else
	{
   #endif
		//Save backbuffer before overlay stuff is written.
		//if(!FCEUI_EmulationPaused())
		memcpy(XBackBuf, XBuf, 256*256);

		//Some messages need to be displayed before the avi is dumped
		//DrawMessage(true);

		#ifdef _S9XLUA_H
		//Lua gui should draw before the avi is dumped.
		FCEU_LuaGui(XBuf);
		#endif


		if(GameInfo->type==GIT_VSUNI)
			FCEU_VSUniDraw(XBuf);

		FCEU_DrawSaveStates(XBuf);
		//FCEU_DrawMovies(XBuf);
		//FCEU_DrawLagCounter(XBuf);
		FCEU_DrawNTSCControlBars(XBuf);
		//FCEU_DrawRecordingStatus(XBuf);
	//}

	DrawMessage(false);

	//if(FCEUD_ShouldDrawInputAids())
	FCEU_DrawInput(XBuf);
}
#endif

void FCEU_DispMessage(const char *format, int disppos=0, ...)
{
	va_list ap;

	va_start(ap,disppos);
	vsnprintf(guiMessage.errmsg,sizeof(guiMessage.errmsg),format,ap);
	va_end(ap);

	guiMessage.howlong = 180;
	guiMessage.isMovieMessage = false;

	guiMessage.linesFromBottom = disppos;
}

void FCEU_ResetMessages()
{
	guiMessage.howlong = 0;
	guiMessage.isMovieMessage = false;
	guiMessage.linesFromBottom = 0;
}


#if 0
int GetScreenPixelPalette(int x, int y, bool usebackup) {

	if (((x < 0) || (x > 255)) || ((y < 0) || (y > 255)))
		return -1;

	if (usebackup)
		return XBackBuf[(y*256)+x] & 0x3f;
	else
		return XBuf[(y*256)+x] & 0x3f;

}
#endif
