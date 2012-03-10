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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#else
#define lseek fseek
#endif

#include "types.h"
#include "file.h"
#include "myendian.h"
#include "memory.h"
#include "driver.h"
#include "general.h"

typedef struct {
     uint8 *data;
     uint32 size;
     uint32 location;
} MEMWRAP;

void ApplyIPS(FILE *ips, MEMWRAP *dest)
{
	uint8 header[5];
	uint32 count=0;

#ifdef FCEU_LOG
	FCEU_printf(" Applying IPS...\n");
#endif
	if(fread(header,1,5,ips)!=5)
	{
		fclose(ips);
		return;
	}
	if(memcmp(header,"PATCH",5))
	{
		fclose(ips);
		return;
	}

	while(fread(header,1,3,ips)==3)
	{
		uint32 offset=(header[0]<<16)|(header[1]<<8)|header[2];
		uint16 size;

		if(!memcmp(header,"EOF",3))
		{
			#ifdef FCEU_LOG
			FCEU_printf(" IPS EOF:  Did %d patches\n\n",count);
			#endif
			fclose(ips);
			return;
		}

		size=fgetc(ips)<<8;
		size|=fgetc(ips);
		if(!size)  /* RLE */
		{
			uint8 *start;
			uint8 b;
			size=fgetc(ips)<<8;
			size|=fgetc(ips);

			/*FCEU_printf("  Offset: %8d  Size: %5d RLE\n",offset,size);*/

			if((offset+size)>dest->size)
			{
				uint8 *tmp;

				/* Probably a little slow.*/
				tmp=(uint8 *)realloc(dest->data,offset+size);
				if(!tmp)
				{
					#ifdef FCEU_LOG
					FCEU_printf("  Oops.  IPS patch %d(type RLE) goes beyond end of file.  Could not allocate memory.\n",count);
					#endif
					fclose(ips);
					return;
				}
				dest->size=offset+size;
				dest->data=tmp;
				memset(dest->data+dest->size,0,offset+size-dest->size);
			}
			b=fgetc(ips);
			start=dest->data+offset;
			do
			{
				*start=b;
				start++;
			} while(--size);
		}
		else    /* Normal patch */
		{
			/*FCEU_printf("  Offset: %8d  Size: %5d\n",offset,size);*/
			if((offset+size)>dest->size)
			{
				uint8 *tmp;

				/* Probably a little slow.*/
				tmp=(uint8 *)realloc(dest->data,offset+size);
				if(!tmp)
				{
					#ifdef FCEU_LOG
					FCEU_printf("  Oops.  IPS patch %d(type normal) goes beyond end of file.  Could not allocate memory.\n",count);
					#endif
					fclose(ips);
					return;
				}
				dest->data=tmp;
				memset(dest->data+dest->size,0,offset+size-dest->size);
			}
			fread(dest->data+offset,1,size,ips);
		}
		count++;
	}
	fclose(ips);
	#ifdef FCEU_LOG
	FCEU_printf(" Hard IPS end!\n");
	#endif
}

static MEMWRAP *MakeMemWrap(void *tz, int type)
{
 MEMWRAP *tmp;

 if(!(tmp=(MEMWRAP *)FCEU_malloc(sizeof(MEMWRAP))))
  goto doret;
 tmp->location=0;
 
 fseek((FILE *)tz,0,SEEK_END);
 tmp->size=ftell((FILE *)tz);
 fseek((FILE *)tz,0,SEEK_SET);
 
 if(!(tmp->data=(uint8*)FCEU_malloc(tmp->size)))
 {
   free(tmp);
   tmp=0;
   goto doret;
 }
 fread(tmp->data,1,tmp->size,(FILE *)tz);

doret:
 fclose((FILE *)tz);
 return tmp;
}

#ifndef __GNUC__
 #define strcasecmp strcmp
#endif


FCEUFILE * FCEU_fopen(const char *path, const char *ipsfn, char *mode, char *ext)
{
	FILE *ipsfile=0;
	FCEUFILE *fceufp;
	void *t;

	if(strchr(mode,'r'))
		ipsfile=fopen(ipsfn,"rb");

	fceufp=(FCEUFILE *)malloc(sizeof(FCEUFILE));

	if((t=fopen(path,"rb")))
	{
		uint32 magic;

		magic=fgetc((FILE *)t);
		magic|=fgetc((FILE *)t)<<8;
		magic|=fgetc((FILE *)t)<<16;

		if(magic!=0x088b1f)   /* Not gzip... */
			fclose((FILE *)t);

	}

	if((t=fopen(path,mode)))
	{
		fseek((FILE *)t,0,SEEK_SET);
		fceufp->type=0;
		fceufp->fp=t;
		if(ipsfile)
		{
			if(!(fceufp->fp=MakeMemWrap(t,0)))
			{
				free(fceufp);
				return(0);
			}
			fceufp->type=3;
			ApplyIPS(ipsfile,(MEMWRAP *)fceufp->fp);
		}
		return(fceufp);
	}

	free(fceufp);
	return 0;
}

int FCEU_fclose(FCEUFILE *fp)
{
  fclose((FILE *)fp->fp);
 free(fp);
 fp=0;
 return 1;
}

uint64 FCEU_fread(void *ptr, size_t size, size_t nmemb, FCEUFILE *fp)
{
 return fread(ptr,size,nmemb,(FILE *)fp->fp);
}

uint64 FCEU_fwrite(void *ptr, size_t size, size_t nmemb, FCEUFILE *fp)
{
  return fwrite(ptr,size,nmemb,(FILE *)fp->fp);
}

int FCEU_fseek(FCEUFILE *fp, long offset, int whence)
{
  return fseek((FILE *)fp->fp,offset,whence);
}

int FCEU_read16le(uint16 *val, FCEUFILE *fp)
{
 uint8 t[2];

  if(fread(t,1,2,(FILE *)fp->fp)!=2) return(0);
 *val=t[0]|(t[1]<<8);
 return(1);
}

int FCEU_read32le(uint32 *Bufo, FCEUFILE *fp)
{
  return read32le(Bufo,(FILE *)fp->fp);
}

int FCEU_fgetc(FCEUFILE *fp)
{
  return fgetc((FILE *)fp->fp);
}

uint64 FCEU_fgetsize(FCEUFILE *fp)
{
  long t,r;
  t=ftell((FILE *)fp->fp);
  fseek((FILE *)fp->fp,0,SEEK_END);
  r=ftell((FILE *)fp->fp);
  fseek((FILE *)fp->fp,t,SEEK_SET);
  return r;
}
