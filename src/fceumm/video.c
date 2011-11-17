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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <zlib.h>

#include "types.h"
#include "video.h"
#include "fceu.h"
#include "general.h"
#include "memory.h"
#include "crc32.h"
#include "state.h"
#include "palette.h"
#include "input.h"
#include "vsuni.h"

uint8 *XBuf=NULL;

int FCEU_InitVirtualVideo(void)
{
	if(!XBuf)    /* Some driver code may allocate XBuf externally. */
		/* 256 bytes per scanline, * 240 scanline maximum, +8 for alignment,
		 */
		if(!(XBuf= (uint8*) (FCEU_malloc(256 * 256 + 8))))
			return 0;

	if(sizeof(uint8*)==4)
	{
		uint32 m;
		m=(uint32)XBuf;
		m=(4-m)&3;
		XBuf+=m;
	}
	memset(XBuf,128,256*256); //*240);
	return 1;
}

static int howlong;
static char errmsg[65];

void FCEU_DispMessage(char *format, ...)
{
 va_list ap;

 va_start(ap,format);
 vsprintf(errmsg,format,ap);
 va_end(ap);

 howlong=180;
}

void FCEU_ResetMessages(void)
{
 howlong=0;
}
