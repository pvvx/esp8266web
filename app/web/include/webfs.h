/*********************************************************************
 *
 *               WEB File System
 *
 *********************************************************************
 * FileName:        WEBFS.h
 ********************************************************************/
#ifndef __WEBFS1_H
#define __WEBFS1_H

#include "user_config.h"

#define WEBFS_USE_SPI_FLASH

#define WEBFS_DISK_ADDR_MINFLASH      0x0a000 // SDK < 0.9.5 = 0x1E000, SDK 0.9.5 = 0x0a000
#define WEBFS_DISK_ADDR_BIGFLASH      0x80000

#define WEBFS_DISK_ID 	0x42455746 // заголовок WEBFiles.bin
#define WEBFS_DISK_VER 	0x0001	   // версия

#define MAX_FILE_NAME_SIZE  64

#ifndef MAX_WEBFS_OPENFILES
  #define MAX_WEBFS_OPENFILES  31
#endif

/****************************************************************************
  Section:
        Type Definitions
  ***************************************************************************/
        #define WEBFS_FLAG_ISZIPPED             0x0001  // Indicates a file is compressed with GZIP compression
        #define WEBFS_FLAG_HASINDEX             0x0002  // Indicates a file has an associated index of dynamic variables
        #define WEBFS_INVALID                    0xffffffff   // Indicates a position pointer is invalid
        #define WEBFS_INVALID_FAT                0xffff       // Indicates an invalid FAT cache
		typedef uint32 WEBFS_PTR;       	// WEBFS Pointers are currently uint32s
		typedef uint8 WEBFS_HANDLE;     	// WEBFS Handles are currently stored as uint8s
        #define WEBFS_INVALID_HANDLE     0xff                 // Indicates that a handle is not valid


        // Stores each file handle's information
        // Handles are free when addr = WEBFS_INVALID
        typedef struct
        {
                WEBFS_PTR addr;          // Current address in the file system
                uint32 bytesRem;         // How many uint8s remain in this file
#ifdef USE_MAX_IRAM
                uint32 fatID;            // ID of which file in the FAT was accessed
#else
                uint16 fatID;            // ID of which file in the FAT was accessed
#endif
        } WEBFS_STUB;

        // Indicates the method for WEBFSSeek
        typedef enum
        {
                WEBFS_SEEK_START         = 0,    // Seek forwards from the front of the file
                WEBFS_SEEK_END,                  // Seek backwards from the end of the file
                WEBFS_SEEK_FORWARD,              // Seek forward from the current position
                WEBFS_SEEK_REWIND                // See backwards from the current position
        } WEBFS_SEEK_MODE;


        typedef struct __attribute__((packed))
        {
                uint32 id;
                uint16 ver;
                uint16 numFiles;
                uint32 disksize;
        } WEBFS_DISK_HEADER ;

        typedef struct  __attribute__((packed))
        {
                uint32 blksize;             // Length of file data - headlen
                uint16 headlen;             // headlen (Length of File Name + 0)
                uint16 flags;               // Flags for this file
        } WEBFS_FHEADER;

        // Stores the data for an WEBFS1 FAT record
        typedef struct
        {
                uint32 string;           // Pointer to the file name
                uint32 data;             // Address of the file data
                uint32 len;              // Length of file data
#ifdef USE_MAX_IRAM
                uint32  flags;           // Flags for this file
#else
                uint16  flags;           // Flags for this file
#endif
        } WEBFS_FAT_RECORD ;

/****************************************************************************
  Section:
        Function Definitions
  ***************************************************************************/

void WEBFSInit(void);

WEBFS_HANDLE WEBFSOpen(uint8* cFile);
//WEBFS_HANDLE WEBFSOpenID(uint16 hFatID);
void WEBFSClose(WEBFS_HANDLE hWEBFS) ICACHE_FLASH_ATTR;

//bool WEBFSGet(WEBFS_HANDLE hWEBFS, uint8* cData);
uint16 WEBFSGetArray(WEBFS_HANDLE hWEBFS, uint8* cData, uint16 wLen);
//bool WEBFSGetLong(WEBFS_HANDLE hWEBFS, uint32* ul);
//bool WEBFSSeek(WEBFS_HANDLE hWEBFS, uint32 dwOffset, WEBFS_SEEK_MODE tMode);

//WEBFS_HANDLE WEBFSFormat(void);
//void WEBFSPutEnd(bool final);
//void WEBFSPutArray(WEBFS_HANDLE hWEBFS, uint8* cData, uint16 wLen);

//uint32 WEBFSGetTimestamp(WEBFS_HANDLE hWEBFS);
//uint32 WEBFSGetMicrotime(WEBFS_HANDLE hWEBFS);
uint16 WEBFSGetFlags(WEBFS_HANDLE hWEBFS);
uint32 WEBFSGetSize(WEBFS_HANDLE hWEBFS);
uint32 WEBFSGetBytesRem(WEBFS_HANDLE hWEBFS);
WEBFS_PTR WEBFSGetStartAddr(WEBFS_HANDLE hWEBFS);
WEBFS_PTR WEBFSGetEndAddr(WEBFS_HANDLE hWEBFS);
bool WEBFSGetFilename(WEBFS_HANDLE hWEBFS, uint8* cName, uint16 wLen);
uint32 WEBFSGetPosition(WEBFS_HANDLE hWEBFS);
//uint16 WEBFSGetID(WEBFS_HANDLE hWEBFS);
uint32 WEBFS_max_size(void) ICACHE_FLASH_ATTR;
uint32 WEBFS_curent_size(void) ICACHE_FLASH_ATTR;
uint32 WEBFS_base_addr(void) ICACHE_FLASH_ATTR;

// Alias of WEBFSGetPosition
#define WEBFSTell(a)     WEBFSGetPosition(a)

#ifdef USE_MAX_IRAM
extern int isWEBFSLocked; // Allows the WEBFS to be locked altogether
extern uint32 numFiles;
#else
extern bool isWEBFSLocked; // Allows the WEBFS to be locked altogether
extern uint16 numFiles;
#endif

extern WEBFS_FAT_RECORD fatCache;
extern WEBFS_STUB WEBFSStubs[MAX_WEBFS_OPENFILES+1];
extern uint32 disk_base_addr;

#endif
