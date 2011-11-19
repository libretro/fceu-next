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

/*  TODO: Add (better) file io error checking */
/*  TODO: Change save state file format. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "sound.h"
#include "myendian.h"
#include "fds.h"
#include "general.h"
#include "state.h"
#include "memory.h"
#include "ppu.h"


static void (*SPreSave)(void);
static void (*SPostSave)(void);

static SFORMAT SFMDATA[64];
static int SFEXINDEX;

#define RLSB     FCEUSTATE_RLSB  //0x80000000

#define FCEU_VERSION_NUMERIC 21040

extern SFORMAT FCEUPPU_STATEINFO[];
extern SFORMAT FCEUSND_STATEINFO[];
extern SFORMAT FCEUCTRL_STATEINFO[];

SFORMAT SFCPU[]={
 { &X.PC, 2|RLSB, "PC\0"},
 { &X.A, 1, "A\0\0"},
 { &X.P, 1, "P\0\0"},
 { &X.X, 1, "X\0\0"},
 { &X.Y, 1, "Y\0\0"},
 { &X.S, 1, "S\0\0"},
#ifdef COPYFAMI
 { RAM, 0x4000, "RAM"},
#else
 { RAM, 0x800, "RAM"},
#endif
 { 0 }
};

SFORMAT SFCPUC[]={
 { &X.jammed, 1, "JAMM"},
 { &X.IRQlow, 4|RLSB, "IQLB"},
 { &X.tcount, 4|RLSB, "ICoa"},
 { &X.count,  4|RLSB, "ICou"},
 { &timestampbase, sizeof(timestampbase) | RLSB, "TSBS"},
 { 0 }
};

static int SubWrite(FILE *st, SFORMAT *sf)
{
	uint32 acc=0;

	while(sf->v)
	{
		if(sf->s==~0)    /* Link to another struct.  */
		{
			uint32 tmp;

			if(!(tmp=SubWrite(st,(SFORMAT *)sf->v)))
				return(0);
			acc+=tmp;
			sf++;
			continue;
		}

		acc+=8;      /* Description + size */
		acc+=sf->s&(~RLSB);

		if(st)      /* Are we writing or calculating the size of this block? */
		{
			fwrite(sf->desc,1,4,st);
			write32le(sf->s&(~RLSB),st);

#ifndef LSB_FIRST
			if(sf->s&RLSB)
				FlipByteOrder(sf->v,sf->s&(~RLSB));
#endif

			fwrite((uint8 *)sf->v,1,sf->s&(~RLSB),st);
			/* Now restore the original byte order. */
#ifndef LSB_FIRST
			if(sf->s&RLSB)
				FlipByteOrder(sf->v,sf->s&(~RLSB));
#endif
		}
		sf++;
	}

	return(acc);
}

static int WriteStateChunk(FILE *st, int type, SFORMAT *sf)
{
	int bsize;

	fputc(type,st);

	bsize=SubWrite(0,sf);
	write32le(bsize,st);

	if(!SubWrite(st,sf))
		return(0);
	return (bsize+5);
}

static SFORMAT *CheckS(SFORMAT *sf, uint32 tsize, char *desc)
{
	while(sf->v)
	{
		if(sf->s==~0)    /* Link to another SFORMAT structure. */
		{
			SFORMAT *tmp;
			if((tmp= CheckS((SFORMAT *)sf->v, tsize, desc) ))
				return(tmp);
			sf++;
			continue;
		}
		if(!memcmp(desc,sf->desc,4))
		{
			if(tsize!=(sf->s&(~RLSB)))
				return(0);
			return(sf);
		}
		sf++;
	}
	return(0);
}

static int ReadStateChunk(FILE *st, SFORMAT *sf, int size)
{
	SFORMAT *tmp;
	int temp;
	temp=ftell(st);

	while(ftell(st)<temp+size)
	{
		uint32 tsize;
		char toa[4];
		if(fread(toa,1,4,st)<=0)
			return 0;

		read32le(&tsize,st);

		if((tmp=CheckS(sf,tsize,toa)))
		{
			fread((uint8 *)tmp->v,1,tmp->s&(~RLSB),st);

#ifndef LSB_FIRST
			if(tmp->s&RLSB)
				FlipByteOrder(tmp->v,tmp->s&(~RLSB));
#endif
		}
		else
			fseek(st,tsize,SEEK_CUR);
	}
	return 1;
}

static int ReadStateChunks(FILE *st, int32 totalsize)
{
	int t;
	uint32 size;
	int ret=1;

	while(totalsize > 0)
	{
		t=fgetc(st);
		if(t==EOF) break;
		if(!read32le(&size,st)) break;
		totalsize -= size + 5;

		switch(t)
		{
			case 1:if(!ReadStateChunk(st,SFCPU,size)) ret=0;break;
			case 2:if(!ReadStateChunk(st,SFCPUC,size)) ret=0;
				       else
				       {
					       X.mooPI=X.P;  // Quick and dirty hack.
				       }
				       break;
			case 3:if(!ReadStateChunk(st,FCEUPPU_STATEINFO,size)) ret=0;break;
			case 4:if(!ReadStateChunk(st,FCEUCTRL_STATEINFO,size)) ret=0;break;
			case 5:if(!ReadStateChunk(st,FCEUSND_STATEINFO,size)) ret=0;break;
			case 0x10:if(!ReadStateChunk(st,SFMDATA,size)) ret=0;
					  break;
			default: if(fseek(st,size,SEEK_CUR)<0) goto endo;break;
		}
	}
endo:
	return ret;
}


static int CurrentState=0;
extern int geniestage;

static int FCEUSS_SaveFP(FILE *st)
{
	static uint32 totalsize;
	static uint8 header[16]="FCS";

	memset(header+4,0,13);
	header[3]=0xFF;
	FCEU_en32lsb(header + 8, FCEU_VERSION_NUMERIC);
	fwrite(header,1,16,st);
	FCEUPPU_SaveState();
	totalsize=WriteStateChunk(st,1,SFCPU);
	totalsize+=WriteStateChunk(st,2,SFCPUC);
	totalsize+=WriteStateChunk(st,3,FCEUPPU_STATEINFO);
	totalsize+=WriteStateChunk(st,4,FCEUCTRL_STATEINFO);
	totalsize+=WriteStateChunk(st,5,FCEUSND_STATEINFO);
	if(SPreSave) SPreSave();
	totalsize+=WriteStateChunk(st,0x10,SFMDATA);
	if(SPreSave) SPostSave();

	fseek(st,4,SEEK_SET);
	write32le(totalsize,st);
	return(1);
}

void FCEUSS_Save(char *fname)
{
	FILE *st=NULL;
	char *fn;

	if(geniestage==1)
	{
		//FCEU_DispMessage("Cannot save FCS in GG screen.");
		return;
	}

	if(fname)
		st=fopen(fname, "wb");
	else
	{
		st=fopen(fn=FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0),"wb");
		free(fn);
	}

	if(st == NULL)
	{
		//FCEU_DispMessage("State %d save error.",CurrentState);
		return;
	}

	FCEUSS_SaveFP(st);

	fclose(st);
	//FCEU_DispMessage("State %d saved.",CurrentState);
}

static int FCEUSS_LoadFP(FILE *st)
{
	int x;
	uint8 header[16];
	int stateversion;

	fread(&header,1,16,st);
	if(memcmp(header,"FCS",3))
		return(0);

	if(header[3] == 0xFF)
		stateversion = FCEU_de32lsb(header + 8);
	else
		stateversion = header[3] * 100;

	x = ReadStateChunks(st,*(uint32*)(header+4));

	if(stateversion < 9500)
		X.IRQlow=0;

	if(GameStateRestore)
		GameStateRestore(stateversion);

	if(x)
	{
		FCEUPPU_LoadState(stateversion);
		FCEUSND_LoadState(stateversion);
	}
	return(x);
}

int FCEUSS_Load(char *fname)
{
	FILE *st;
	char *fn;

	if(fname)
		st=fopen(fname, "rb");
	else
	{
		st=fopen(fn=FCEU_MakeFName(FCEUMKF_STATE,CurrentState,fname),"rb");
		free(fn);
	}

	if(st == NULL || geniestage == 1)
	{
		//FCEU_DispMessage("State %d load error.",CurrentState);
		return(0);
	}

	int ret = FCEUSS_LoadFP(st);
	fclose(st);
	if(ret)
		return 1;
	else
		return 0;
}

void ResetExState(void (*PreSave)(void), void (*PostSave)(void))
{
	int x;
	for(x=0;x<SFEXINDEX;x++)
	{
		if(SFMDATA[x].desc)
			free(SFMDATA[x].desc);
	}
	SPreSave = PreSave;
	SPostSave = PostSave;
	SFEXINDEX=0;
}

void AddExState(void *v, uint32 s, int type, char *desc)
{
	if(desc)
	{
		SFMDATA[SFEXINDEX].desc=(char *)FCEU_malloc(strlen(desc)+1);
		strcpy(SFMDATA[SFEXINDEX].desc,desc);
	}
	else
		SFMDATA[SFEXINDEX].desc=0;

	SFMDATA[SFEXINDEX].v=v;
	SFMDATA[SFEXINDEX].s=s;
	if(type) SFMDATA[SFEXINDEX].s|=RLSB;
	if(SFEXINDEX<63) SFEXINDEX++;
	SFMDATA[SFEXINDEX].v=0;    // End marker.
}
