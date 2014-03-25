/*
 * File cdaccess.c - Read existing iso9660 image from OS/2 CDROM drive 
 *                   Used for multisession support.
 *
 * Written by Robert Lalla (1999).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
 */

#define  INCL_DOSDEVIOCTL
#include <os2emx.h>

#include "config.h"
#include "mkisofs.h"

static HFILE cd_handle;
static char  use_readlong = 0;

/*---------------------------------------------------*/
/* open OS/2 CD drive for absolute access            */ 
/*---------------------------------------------------*/
int scsidev_open(char *path)
{
   ULONG rc, action;
   ULONG open_mode = OPEN_ACCESS_READONLY       | OPEN_SHARE_DENYNONE |
                     OPEN_FLAGS_FAIL_ON_ERROR   | OPEN_FLAGS_DASD;
   ULONG open_flag = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;

   /* C library open() doesn't allow DASD mode */
   rc = DosOpen (path,&cd_handle,&action,0,0,open_flag,open_mode,NULL);
   if (rc != 0)
   {  if (verbose > 0)
      {  fprintf(stderr,"CD drive %s open error (OS/2 rc=0x%x) \n",path,rc);
      }
      return -1;  /* device open failed */
   }
   return 0;      /* device is open */
}


/*---------------------------------------------------*/
/* read data sectors from previous session           */
/*---------------------------------------------------*/
int readsecs(int startsecno, void *buffer, int sectorcount)
{
   ULONG rc, newPos, bytesRead, par_len, dat_len, i; 
   UCHAR ioctl_rawbuf[2352];
   #pragma pack(1)
   struct { UCHAR  ID_code[4];
            UCHAR  address_mode;
            USHORT transfer_count;
            ULONG  start_sector;
            UCHAR  reserved[3];} ioctl_cmdbuf 
                  = {{'C','D','0','1'},0,0,0,{0,0,0}}; 
   #pragma pack()


   /* tell us about read position */
   if (verbose > 1)
   {  fprintf(stderr,
              "Reading previous session: sector=%i, count=%i \n",
              startsecno,sectorcount);
   }

   /* if image file specified -> use normal C file i/o library */
   if (in_image != NULL)
   {  if (lseek(fileno(in_image),startsecno*SECTOR_SIZE,0) == -1) 
      {  return 0;  /* return read error */
      }
      bytesRead = read(fileno(in_image),buffer,sectorcount*SECTOR_SIZE);
      return bytesRead; /* read completed */
   }
 
   /* first try DosRead to read device, usually working within first session */
   if (use_readlong == 0)
   {  rc = DosSetFilePtr(cd_handle,startsecno*SECTOR_SIZE,FILE_BEGIN,&newPos);
      if ((rc == 0) && (newPos == startsecno*SECTOR_SIZE))
      {  rc = DosRead(cd_handle,buffer,sectorcount*SECTOR_SIZE,&bytesRead);
         if ((rc == 0) && (bytesRead == sectorcount*SECTOR_SIZE))
         {  return bytesRead;    /* read completed */
         }
      }
      /* read error occurred, switch to LongRead */
      if (verbose > 0)
      {  fprintf(stderr,
                 "CD read error at sector %i, trying LongRead \n",
                 startsecno);
      }
      use_readlong = -1;
   }

   /* use IOCtl(ReadLong) function if normal read caused trouble */ 
   if (use_readlong != 0)
   {  for (i=0; i<sectorcount; i++)     /* read single sectors */
      {  par_len = sizeof(ioctl_cmdbuf);
         dat_len = sizeof(ioctl_rawbuf);
         ioctl_cmdbuf.transfer_count = 1;
         ioctl_cmdbuf.start_sector   = startsecno+i;
         rc = DosDevIOCtl(cd_handle,IOCTL_CDROMDISK,CDROMDISK_READLONG, 
                 &ioctl_cmdbuf,par_len,&par_len,&ioctl_rawbuf,dat_len,&dat_len);
         if (rc != 0)
         {  if (verbose > 0)
            {  fprintf(stderr,
                       "CD read error at sector %i (OS/2 rc=0x%x) \n",
                       startsecno+i,rc);
            }
            return i*SECTOR_SIZE;   /* return read error */
         }   
         /* convert raw sector to cooked */
         memcpy(buffer+i*SECTOR_SIZE, &ioctl_rawbuf[24], SECTOR_SIZE);
      }
      return i*SECTOR_SIZE;  /* read completed */
   }
}
